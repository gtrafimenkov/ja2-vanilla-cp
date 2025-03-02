// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/Environment.h"

#include "Laptop/AIMMembers.h"
#include "Laptop/BobbyR.h"
#include "Laptop/InsuranceContract.h"
#include "Laptop/Mercs.h"
#include "Macro.h"
#include "SGP/Random.h"
#include "SGP/SoundMan.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEvents.h"
#include "Strategic/Quests.h"
#include "Strategic/StrategicEventHandler.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMercHandler.h"
#include "Tactical/MercHiring.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "TileEngine/AmbientControl.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/RenderWorld.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/SoundControl.h"

// effects whether or not time of day effects the lighting.  Underground
// maps have an ambient light level that is saved in the map, and doesn't
// change.
BOOLEAN gfBasement = FALSE;
BOOLEAN gfCaves = FALSE;

/*
#define		DAWNLIGHT_START
( 5 * 60 ) #define		DAWN_START
( 6 * 60 ) #define   DAY_START
( 8 * 60 ) #define		TWILLIGHT_START
( 19 * 60 ) #define		DUSK_START
( 20 * 60 ) #define   NIGHT_START
( 22 * 60 )
*/
#define DAWN_START (6 * 60 + 47)    // 6:47AM
#define DAY_START (7 * 60 + 5)      // 7:05AM
#define DUSK_START (20 * 60 + 57)   // 8:57PM
#define NIGHT_START (21 * 60 + 15)  // 9:15PM

#define DAWN_TO_DAY (DAY_START - DAWN_START)
#define DAY_TO_DUSK (DUSK_START - DAY_START)
#define DUSK_TO_NIGHT (NIGHT_START - DUSK_START)
#define NIGHT_TO_DAWN (24 * 60 - NIGHT_START + DAWN_START)

uint32_t guiEnvWeather = 0;
uint32_t guiRainLoop = NO_SAMPLE;

// frame cues for lightning
uint8_t ubLightningTable[3][10][2] = {
    {{0, 15}, {1, 0}, {2, 0}, {3, 6}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}},

    {{0, 15}, {1, 0}, {2, 0}, {3, 6}, {4, 0}, {5, 15}, {6, 0}, {7, 6}, {8, 0}, {9, 0}},

    {{0, 15}, {1, 0}, {2, 15}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}}};

enum Temperatures { COOL, WARM, HOT };

enum TemperatureEvents {
  TEMPERATURE_DESERT_COOL,
  TEMPERATURE_DESERT_WARM,
  TEMPERATURE_DESERT_HOT,
  TEMPERATURE_GLOBAL_COOL,
  TEMPERATURE_GLOBAL_WARM,
  TEMPERATURE_GLOBAL_HOT,
};

#define DESERT_WARM_START (8 * 60)
#define DESERT_HOT_START (9 * 60)
#define DESERT_HOT_END (17 * 60)
#define DESERT_WARM_END (19 * 60)

#define GLOBAL_WARM_START (9 * 60)
#define GLOBAL_HOT_START (12 * 60)
#define GLOBAL_HOT_END (14 * 60)
#define GLOBAL_WARM_END (17 * 60)

#define HOT_DAY_LIGHTLEVEL 2

BOOLEAN fTimeOfDayControls = TRUE;
uint32_t guiEnvTime = 0;
uint32_t guiEnvDay = 0;
uint8_t gubEnvLightValue = 0;
BOOLEAN gfDoLighting = FALSE;

uint8_t gubDesertTemperature = 0;
uint8_t gubGlobalTemperature = 0;

static void EnvDoLightning();

// polled by the game to handle time/atmosphere changes from gamescreen
void EnvironmentController(BOOLEAN fCheckForLights) {
  uint32_t uiOldWorldHour;
  uint8_t ubLightAdjustFromWeather = 0;

  // do none of this stuff in the basement or caves
  if (gfBasement || gfCaves) {
    guiEnvWeather &= (~WEATHER_FORECAST_THUNDERSHOWERS);
    guiEnvWeather &= (~WEATHER_FORECAST_SHOWERS);

    if (guiRainLoop != NO_SAMPLE) {
      SoundStop(guiRainLoop);
      guiRainLoop = NO_SAMPLE;
    }
    return;
  }

  if (fTimeOfDayControls) {
    uiOldWorldHour = GetWorldHour();

    // If hour is different
    if (uiOldWorldHour != guiEnvTime) {
      // Hour change....

      guiEnvTime = uiOldWorldHour;
    }

    // ExecuteStrategicEventsUntilTimeStamp( (uint16_t)GetWorldTotalMin( ) );

    // Polled weather stuff...
    // ONly do indooors
    if (!gfBasement && !gfCaves) {
#if 0
			if ( guiEnvWeather & ( WEATHER_FORECAST_THUNDERSHOWERS | WEATHER_FORECAST_SHOWERS ) )
			{
				if ( guiRainLoop == NO_SAMPLE )
				{
					guiRainLoop	= PlayJA2Ambient( RAIN_1, MIDVOLUME, 0 );
				}

				// Do lightning if we want...
				if ( guiEnvWeather & ( WEATHER_FORECAST_THUNDERSHOWERS ) )
				{
					EnvDoLightning( );
				}

			}
			else
			{
				if ( guiRainLoop != NO_SAMPLE )
				{
					SoundStop( guiRainLoop );
					guiRainLoop = NO_SAMPLE;
				}
			}
#endif
    }

    if (gfDoLighting && fCheckForLights) {
      // Adjust light level based on weather...
      ubLightAdjustFromWeather = GetTimeOfDayAmbientLightLevel();

      // ONly do indooors
      if (!gfBasement && !gfCaves) {
        // Rain storms....
#if 0
				if ( guiEnvWeather & ( WEATHER_FORECAST_THUNDERSHOWERS | WEATHER_FORECAST_SHOWERS ) )
				{
					// Thunder showers.. make darker
					if ( guiEnvWeather & ( WEATHER_FORECAST_THUNDERSHOWERS ) )
					{
						ubLightAdjustFromWeather = (uint8_t)(std::min( gubEnvLightValue+2, NORMAL_LIGHTLEVEL_NIGHT ));
					}
					else
					{
						ubLightAdjustFromWeather = (uint8_t)(std::min( gubEnvLightValue+1, NORMAL_LIGHTLEVEL_NIGHT ));
					}
				}
#endif
      }

      LightSetBaseLevel(ubLightAdjustFromWeather);

      // Update Merc Lights since the above function modifies it.
      HandlePlayerTogglingLightEffects(FALSE);

      // Make teams look for all
      // AllTeamsLookForAll( FALSE );

      // Set global light value
      SetRenderFlags(RENDER_FLAG_FULL);
      gfDoLighting = FALSE;
    }
  }
}

void BuildDayLightLevels() {
  uint32_t uiLoop, uiHour;

  /*
  // Dawn; light 12
  AddEveryDayStrategicEvent( EVENT_CHANGELIGHTVAL, DAWNLIGHT_START,
  NORMAL_LIGHTLEVEL_NIGHT - 1 );

  // loop from light 12 down to light 4
  for (uiLoop = 1; uiLoop < 8; uiLoop++)
  {
          AddEveryDayStrategicEvent( EVENT_CHANGELIGHTVAL, DAWN_START + 15 *
  uiLoop,	NORMAL_LIGHTLEVEL_NIGHT - 1 - uiLoop );
  }
  */

  // Transition from night to day
  for (uiLoop = 0; uiLoop < 9; uiLoop++) {
    AddEveryDayStrategicEvent(EVENT_CHANGELIGHTVAL, DAWN_START + 2 * uiLoop,
                              NORMAL_LIGHTLEVEL_NIGHT - 1 - uiLoop);
  }

  // Add events for hot times
  AddEveryDayStrategicEvent(EVENT_TEMPERATURE_UPDATE, DESERT_WARM_START, TEMPERATURE_DESERT_WARM);
  AddEveryDayStrategicEvent(EVENT_TEMPERATURE_UPDATE, DESERT_HOT_START, TEMPERATURE_DESERT_HOT);
  AddEveryDayStrategicEvent(EVENT_TEMPERATURE_UPDATE, DESERT_HOT_END, TEMPERATURE_DESERT_WARM);
  AddEveryDayStrategicEvent(EVENT_TEMPERATURE_UPDATE, DESERT_WARM_END, TEMPERATURE_DESERT_COOL);

  AddEveryDayStrategicEvent(EVENT_TEMPERATURE_UPDATE, GLOBAL_WARM_START, TEMPERATURE_GLOBAL_WARM);
  AddEveryDayStrategicEvent(EVENT_TEMPERATURE_UPDATE, GLOBAL_HOT_START, TEMPERATURE_GLOBAL_HOT);
  AddEveryDayStrategicEvent(EVENT_TEMPERATURE_UPDATE, GLOBAL_HOT_END, TEMPERATURE_GLOBAL_WARM);
  AddEveryDayStrategicEvent(EVENT_TEMPERATURE_UPDATE, GLOBAL_WARM_END, TEMPERATURE_GLOBAL_COOL);

  /*
          // Twilight; light 5
          AddEveryDayStrategicEvent( EVENT_CHANGELIGHTVAL, TWILLIGHT_START,
     NORMAL_LIGHTLEVEL_DAY + 1 );

          // Dusk; loop from light 5 up to 12
          for (uiLoop = 1; uiLoop < 8; uiLoop++)
          {
                  AddEveryDayStrategicEvent( EVENT_CHANGELIGHTVAL, DUSK_START +
     15 * uiLoop, NORMAL_LIGHTLEVEL_DAY + 1 + uiLoop );
          }
  */

  // Transition from day to night
  for (uiLoop = 0; uiLoop < 9; uiLoop++) {
    AddEveryDayStrategicEvent(EVENT_CHANGELIGHTVAL, DUSK_START + 2 * uiLoop,
                              NORMAL_LIGHTLEVEL_DAY + 1 + uiLoop);
  }

  // Set up the scheduling for turning lights on and off based on the various
  // types.
  uiHour = NIGHT_TIME_LIGHT_START_HOUR == 24 ? 0 : NIGHT_TIME_LIGHT_START_HOUR;
  AddEveryDayStrategicEvent(EVENT_TURN_ON_NIGHT_LIGHTS, uiHour * 60, 0);
  uiHour = NIGHT_TIME_LIGHT_END_HOUR == 24 ? 0 : NIGHT_TIME_LIGHT_END_HOUR;
  AddEveryDayStrategicEvent(EVENT_TURN_OFF_NIGHT_LIGHTS, uiHour * 60, 0);
  uiHour = PRIME_TIME_LIGHT_START_HOUR == 24 ? 0 : PRIME_TIME_LIGHT_START_HOUR;
  AddEveryDayStrategicEvent(EVENT_TURN_ON_PRIME_LIGHTS, uiHour * 60, 0);
  uiHour = PRIME_TIME_LIGHT_END_HOUR == 24 ? 0 : PRIME_TIME_LIGHT_END_HOUR;
  AddEveryDayStrategicEvent(EVENT_TURN_OFF_PRIME_LIGHTS, uiHour * 60, 0);
}

void BuildDayAmbientSounds() {
  int32_t cnt;

  // Add events!
  for (cnt = 0; cnt < gsNumAmbData; cnt++) {
    switch (gAmbData[cnt].ubTimeCatagory) {
      case AMB_TOD_DAWN:
        AddSameDayRangedStrategicEvent(EVENT_AMBIENT, DAWN_START, DAWN_TO_DAY, cnt);
        break;
      case AMB_TOD_DAY:
        AddSameDayRangedStrategicEvent(EVENT_AMBIENT, DAY_START, DAY_TO_DUSK, cnt);
        break;
      case AMB_TOD_DUSK:
        AddSameDayRangedStrategicEvent(EVENT_AMBIENT, DUSK_START, DUSK_TO_NIGHT, cnt);
        break;
      case AMB_TOD_NIGHT:
        AddSameDayRangedStrategicEvent(EVENT_AMBIENT, NIGHT_START, NIGHT_TO_DAWN, cnt);
        break;
    }
  }

  guiRainLoop = NO_SAMPLE;
}

void ForecastDayEvents() {
  uint32_t uiOldDay;
  uint32_t uiStartTime, uiEndTime;
  uint8_t ubStormIntensity;
  //	uint32_t cnt;

  // Get current day and see if different
  if ((uiOldDay = GetWorldDay()) != guiEnvDay) {
    // It's a new day, forecast weather
    guiEnvDay = uiOldDay;

    // Set light level changes
    // BuildDayLightLevels();

    // Build ambient sound queues
    BuildDayAmbientSounds();

    // Build weather....

    // ATE: Don't forecast if start of game...
    if (guiEnvDay > 1) {
      // Should it rain...?
      if (Random(100) < 20) {
        // Add rain!
        // Between 6:00 and 10:00
        uiStartTime = (uint32_t)(360 + Random(1080));
        // Between 5 - 15 miniutes
        uiEndTime = uiStartTime + (5 + Random(10));

        ubStormIntensity = 0;

        // Randomze for a storm!
        if (Random(10) < 5) {
          ubStormIntensity = 1;
        }

        // ATE: Disable RAIN!
        //			AddSameDayRangedStrategicEvent( EVENT_RAINSTORM,
        // uiStartTime, uiEndTime - uiStartTime, ubStormIntensity );

        // AddSameDayStrategicEvent( EVENT_BEGINRAINSTORM, uiStartTime,
        // ubStormIntensity ); AddSameDayStrategicEvent( EVENT_ENDRAINSTORM,
        // uiEndTime, 0 );
      }
    }
  }
}

static void EnvEnableTOD() { fTimeOfDayControls = TRUE; }

static void EnvDisableTOD() { fTimeOfDayControls = FALSE; }

static void EnvDoLightning() {
  static uint32_t uiCount = 0, uiIndex = 0, uiStrike = 0, uiFrameNext = 1000;
  static uint8_t ubLevel = 0, ubLastLevel = 0;

  if (gfPauseDueToPlayerGamePause) {
    return;
  }

  uiCount++;
  if (uiCount >= (uiFrameNext + 10)) {
    uiCount = 0;
    uiIndex = 0;
    ubLevel = 0;
    ubLastLevel = 0;

    uiStrike = Random(3);
    uiFrameNext = 1000 + Random(1000);
  } else if (uiCount >= uiFrameNext) {
    if (uiCount == uiFrameNext) {
      // EnvStopCrickets();
      PlayJA2Ambient(Random(2) == 0 ? LIGHTNING_1 : LIGHTNING_2, HIGHVOLUME, 1);
    }

    while (uiCount > ((uint32_t)ubLightningTable[uiStrike][uiIndex][0] + uiFrameNext)) uiIndex++;

    ubLastLevel = ubLevel;
    ubLevel = ubLightningTable[uiStrike][uiIndex][1];

    // ATE: Don't modify if scrolling!
    if (!gfScrollPending && !g_scroll_inertia) {
      if (ubLastLevel != ubLevel) {
        if (ubLevel > ubLastLevel) {
          LightAddBaseLevel(ubLevel - ubLastLevel);
          if (ubLevel > 0) RenderSetShadows(TRUE);
        } else {
          LightSubtractBaseLevel(ubLastLevel - ubLevel);
          if (ubLevel > 0) RenderSetShadows(TRUE);
        }
        SetRenderFlags(RENDER_FLAG_FULL);
      }
    }
  }
}

uint8_t GetTimeOfDayAmbientLightLevel() {
  if (SectorTemperature(GetWorldMinutesInDay(), gWorldSectorX, gWorldSectorY, gbWorldSectorZ) ==
      HOT) {
    return (HOT_DAY_LIGHTLEVEL);
  } else {
    return (gubEnvLightValue);
  }
}

void EnvBeginRainStorm(uint8_t ubIntensity) {
  if (!gfBasement && !gfCaves) {
    gfDoLighting = TRUE;

    if (ubIntensity == 1) {
      // Turn on rain storms
      guiEnvWeather |= WEATHER_FORECAST_THUNDERSHOWERS;
    } else {
      guiEnvWeather |= WEATHER_FORECAST_SHOWERS;
    }
  }
}

void EnvEndRainStorm() {
  gfDoLighting = TRUE;
  guiEnvWeather &= (~WEATHER_FORECAST_THUNDERSHOWERS);
  guiEnvWeather &= (~WEATHER_FORECAST_SHOWERS);
}

void TurnOnNightLights() {
  FOR_EACH_LIGHT_SPRITE(l) {
    if (l->uiFlags & LIGHT_NIGHTTIME && !(l->uiFlags & (LIGHT_SPR_ON | MERC_LIGHT))) {
      LightSpritePower(l, TRUE);
    }
  }
}

void TurnOffNightLights() {
  FOR_EACH_LIGHT_SPRITE(l) {
    if (l->uiFlags & LIGHT_NIGHTTIME && l->uiFlags & LIGHT_SPR_ON && !(l->uiFlags & MERC_LIGHT)) {
      LightSpritePower(l, FALSE);
    }
  }
}

void TurnOnPrimeLights() {
  FOR_EACH_LIGHT_SPRITE(l) {
    if (l->uiFlags & LIGHT_PRIMETIME && !(l->uiFlags & (LIGHT_SPR_ON | MERC_LIGHT))) {
      LightSpritePower(l, TRUE);
    }
  }
}

void TurnOffPrimeLights() {
  FOR_EACH_LIGHT_SPRITE(l) {
    if (l->uiFlags & LIGHT_PRIMETIME && l->uiFlags & LIGHT_SPR_ON && !(l->uiFlags & MERC_LIGHT)) {
      LightSpritePower(l, FALSE);
    }
  }
}

void UpdateTemperature(uint8_t ubTemperatureCode) {
  switch (ubTemperatureCode) {
    case TEMPERATURE_DESERT_COOL:
      gubDesertTemperature = 0;
      break;
    case TEMPERATURE_DESERT_WARM:
      gubDesertTemperature = 1;
      break;
    case TEMPERATURE_DESERT_HOT:
      gubDesertTemperature = 2;
      break;
    case TEMPERATURE_GLOBAL_COOL:
      gubGlobalTemperature = 0;
      break;
    case TEMPERATURE_GLOBAL_WARM:
      gubGlobalTemperature = 1;
      break;
    case TEMPERATURE_GLOBAL_HOT:
      gubGlobalTemperature = 2;
      break;
  }
  gfDoLighting = TRUE;
}

int8_t SectorTemperature(uint32_t uiTime, int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ) {
  if (bSectorZ > 0) {
    // cool underground
    return (0);
  } else if (IsSectorDesert(sSectorX, sSectorY))  // is desert
  {
    return (gubDesertTemperature);
  } else {
    return (gubGlobalTemperature);
  }
}
