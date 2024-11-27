#include "Utils/TimerControl.h"

#include <SDL.h>
#include <stdexcept>

#include "Macro.h"
#include "SGP/Debug.h"
#include "Strategic/MapScreen.h"
#include "Tactical/HandleItems.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierControl.h"
#include "TileEngine/WorldDef.h"

int32_t giClockTimer = -1;
int32_t giTimerDiag = 0;

uint32_t guiBaseJA2Clock = 0;

static BOOLEAN gfPauseClock = FALSE;

const int32_t giTimerIntervals[NUMTIMERS] = {
    5,     // Tactical Overhead
    20,    // NEXTSCROLL
    200,   // Start Scroll
    200,   // Animate tiles
    1000,  // FPS Counter
    80,    // PATH FIND COUNTER
    150,   // CURSOR TIMER
    250,   // RIGHT CLICK FOR MENU
    300,   // LEFT
    200,   // TARGET REFINE TIMER
    150,   // CURSOR/AP FLASH
    20,    // PHYSICS UPDATE
    100,   // FADE ENEMYS
    20,    // STRATEGIC OVERHEAD
    40,
    500,  // NON GUN TARGET REFINE TIMER
    250,  // IMPROVED CURSOR FLASH
    500,  // 2nd CURSOR FLASH
    400,  // RADARMAP BLINK AND OVERHEAD MAP BLINK SHOUDL BE THE SAME
    10    // Music Overhead
};

// TIMER COUNTERS
int32_t giTimerCounters[NUMTIMERS];

int32_t giTimerAirRaidQuote = 0;
int32_t giTimerAirRaidDiveStarted = 0;
int32_t giTimerAirRaidUpdate = 0;
int32_t giTimerCustomizable = 0;
int32_t giTimerTeamTurnUpdate = 0;

CUSTOMIZABLE_TIMER_CALLBACK gpCustomizableTimerCallback = 0;

// Clock Callback event ID
static SDL_TimerID g_timer;

extern uint32_t guiCompressionStringBaseTime;
extern int32_t giFlashHighlightedItemBaseTime;
extern int32_t giCompatibleItemBaseTime;
extern int32_t giAnimateRouteBaseTime;
extern int32_t giPotHeliPathBaseTime;
extern uint32_t guiSectorLocatorBaseTime;
extern int32_t giCommonGlowBaseTime;
extern int32_t giFlashAssignBaseTime;
extern int32_t giFlashContractBaseTime;
extern uint32_t guiFlashCursorBaseTime;
extern int32_t giPotCharPathBaseTime;

static uint32_t TimeProc(uint32_t const interval, void *) {
  if (!gfPauseClock) {
    guiBaseJA2Clock += BASETIMESLICE;

    for (uint32_t i = 0; i != NUMTIMERS; i++) {
      UPDATECOUNTER(i);
    }

    // Update some specialized countdown timers...
    UPDATETIMECOUNTER(giTimerAirRaidQuote);
    UPDATETIMECOUNTER(giTimerAirRaidDiveStarted);
    UPDATETIMECOUNTER(giTimerAirRaidUpdate);
    UPDATETIMECOUNTER(giTimerTeamTurnUpdate);

    if (gpCustomizableTimerCallback) {
      UPDATETIMECOUNTER(giTimerCustomizable);
    }

#ifndef BOUNDS_CHECKER
    if (fInMapMode) {
      // IN Mapscreen, loop through player's team
      FOR_EACH_IN_TEAM(s, OUR_TEAM) {
        UPDATETIMECOUNTER(s->PortraitFlashCounter);
        UPDATETIMECOUNTER(s->PanelAnimateCounter);
      }
    } else {
      // Set update flags for soldiers
      FOR_EACH_MERC(i) {
        SOLDIERTYPE *const s = *i;
        UPDATETIMECOUNTER(s->UpdateCounter);
        UPDATETIMECOUNTER(s->DamageCounter);
        UPDATETIMECOUNTER(s->BlinkSelCounter);
        UPDATETIMECOUNTER(s->PortraitFlashCounter);
        UPDATETIMECOUNTER(s->AICounter);
        UPDATETIMECOUNTER(s->FadeCounter);
        UPDATETIMECOUNTER(s->NextTileCounter);
        UPDATETIMECOUNTER(s->PanelAnimateCounter);
      }
    }
#endif
  }

  return interval;
}

void InitializeJA2Clock() {
#ifdef CALLBACKTIMER
  SDL_InitSubSystem(SDL_INIT_TIMER);

  // Init timer delays
  for (int32_t i = 0; i != NUMTIMERS; ++i) {
    giTimerCounters[i] = giTimerIntervals[i];
  }

  g_timer = SDL_AddTimer(BASETIMESLICE, TimeProc, 0);
  if (!g_timer) throw std::runtime_error("Could not create timer callback");
#endif
}

void ShutdownJA2Clock() {
#ifdef CALLBACKTIMER
  SDL_RemoveTimer(g_timer);
#endif
}

void PauseTime(BOOLEAN const fPaused) { gfPauseClock = fPaused; }

void SetCustomizableTimerCallbackAndDelay(int32_t const delay,
                                          CUSTOMIZABLE_TIMER_CALLBACK const callback,
                                          BOOLEAN const replace) {
  if (!replace && gpCustomizableTimerCallback) {  // Replace callback but call
                                                  // the current callback first
    gpCustomizableTimerCallback();
  }

  RESETTIMECOUNTER(giTimerCustomizable, delay);
  gpCustomizableTimerCallback = callback;
}

void CheckCustomizableTimer() {
  if (!gpCustomizableTimerCallback) return;
  if (!TIMECOUNTERDONE(giTimerCustomizable, 0)) return;

  /* Set the callback to a temp variable so we can reset the global variable
   * before calling the callback, so that if the callback sets up another
   * instance of the timer, we don't reset it afterwards. */
  CUSTOMIZABLE_TIMER_CALLBACK const callback = gpCustomizableTimerCallback;
  gpCustomizableTimerCallback = 0;
  callback();
}

void ResetJA2ClockGlobalTimers() {
  uint32_t const now = GetJA2Clock();

  guiCompressionStringBaseTime = now;
  giFlashHighlightedItemBaseTime = now;
  giCompatibleItemBaseTime = now;
  giAnimateRouteBaseTime = now;
  giPotHeliPathBaseTime = now;
  guiSectorLocatorBaseTime = now;

  giCommonGlowBaseTime = now;
  giFlashAssignBaseTime = now;
  giFlashContractBaseTime = now;
  guiFlashCursorBaseTime = now;
  giPotCharPathBaseTime = now;
}
