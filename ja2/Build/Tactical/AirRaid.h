#ifndef AIR_RAID_H
#define AIR_RAID_H

#include "SGP/Types.h"

#define AIR_RAID_BEGINNING_GAME 0x00000001
#define AIR_RAID_CAN_RANDOMIZE_TEASE_DIVES 0x00000002

struct AIR_RAID_DEFINITION {
  int16_t sSectorX;
  int16_t sSectorY;
  int16_t sSectorZ;
  int8_t bIntensity;
  uint32_t uiFlags;
  uint8_t ubNumMinsFromCurrentTime;
  uint8_t ubFiller[8];  // XXX HACK000B
};

extern BOOLEAN gfInAirRaid;

// what ari raid mode are we in?
extern uint8_t gubAirRaidMode;

enum AIR_RAID_STATES {
  AIR_RAID_TRYING_TO_START,
  AIR_RAID_START,
  AIR_RAID_LOOK_FOR_DIVE,
  AIR_RAID_BEGIN_DIVE,
  AIR_RAID_DIVING,
  AIR_RAID_END_DIVE,
  AIR_RAID_BEGIN_BOMBING,
  AIR_RAID_BOMBING,
  AIR_RAID_END_BOMBING,
  AIR_RAID_START_END,
  AIR_RAID_END
};

void HandleAirRaid();

BOOLEAN InAirRaid();

BOOLEAN HandleAirRaidEndTurn(uint8_t ubTeam);

// Save the air raid info to the saved game
void SaveAirRaidInfoToSaveGameFile(HWFILE);

// load the air raid info from the saved game
void LoadAirRaidInfoFromSaveGameFile(HWFILE);

void EndAirRaid();

#endif
