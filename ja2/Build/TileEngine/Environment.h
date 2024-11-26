#ifndef _ENVIRONMENT_H_
#define _ENVIRONMENT_H_

#include "SGP/Types.h"

// Make sure you use 24 for end time hours and 0 for start time hours if
// midnight is the hour you wish to use.
#define NIGHT_TIME_LIGHT_START_HOUR 21
#define NIGHT_TIME_LIGHT_END_HOUR 7
#define PRIME_TIME_LIGHT_START_HOUR 21
#define PRIME_TIME_LIGHT_END_HOUR 24

#define WEATHER_FORECAST_SUNNY 0x00000001
#define WEATHER_FORECAST_OVERCAST 0x00000002
#define WEATHER_FORECAST_PARTLYSUNNY 0x00000004
#define WEATHER_FORECAST_DRIZZLE 0x00000008
#define WEATHER_FORECAST_SHOWERS 0x00000010
#define WEATHER_FORECAST_THUNDERSHOWERS 0x00000020

// higher is darker, remember
#define NORMAL_LIGHTLEVEL_NIGHT 12
#define NORMAL_LIGHTLEVEL_DAY 3

void ForecastDayEvents();

void EnvironmentController(BOOLEAN fCheckForLights);

void BuildDayAmbientSounds();
void BuildDayLightLevels();
uint8_t GetTimeOfDayAmbientLightLevel();

void EnvBeginRainStorm(uint8_t ubIntensity);
void EnvEndRainStorm();

extern uint8_t gubEnvLightValue;
extern BOOLEAN gfDoLighting;
extern uint32_t guiEnvWeather;

void TurnOnNightLights();
void TurnOffNightLights();
void TurnOnPrimeLights();
void TurnOffPrimeLights();

// effects whether or not time of day effects the lighting.  Underground
// maps have an ambient light level that is saved in the map, and doesn't
// change.
extern BOOLEAN gfCaves;
extern BOOLEAN gfBasement;

extern int8_t SectorTemperature(uint32_t uiTime, int16_t sSectorX, int16_t sSectorY,
                                int8_t bSectorZ);

extern void UpdateTemperature(uint8_t ubTemperatureCode);

#endif
