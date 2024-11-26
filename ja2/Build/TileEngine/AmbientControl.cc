#include "TileEngine/AmbientControl.h"

#include <cstring>

#include "Directories.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/SoundMan.h"
#include "SGP/StrUtils.h"
#include "Strategic/GameEvents.h"
#include "TileEngine/AmbientTypes.h"
#include "TileEngine/Environment.h"
#include "Utils/SoundControl.h"

AMBIENTDATA_STRUCT gAmbData[MAX_AMBIENT_SOUNDS];
int16_t gsNumAmbData = 0;

static BOOLEAN LoadAmbientControlFile(uint8_t ubAmbientID) try {
  std::string zFilename = FormattedString(AMBIENTDIR "/%d.bad", ubAmbientID);

  AutoSGPFile hFile(FileMan::openForReadingSmart(zFilename.c_str(), true));

  // READ #
  FileRead(hFile, &gsNumAmbData, sizeof(int16_t));

  // LOOP FOR OTHERS
  for (int32_t cnt = 0; cnt < gsNumAmbData; cnt++) {
    FileRead(hFile, &gAmbData[cnt], sizeof(AMBIENTDATA_STRUCT));
    std::string filename = FormattedString(AMBIENTDIR "/%s", gAmbData[cnt].zFilename);
    strcpy(gAmbData[cnt].zFilename, filename.c_str());
  }

  return TRUE;
} catch (...) {
  return FALSE;
}

void StopAmbients() { SoundStopAllRandom(); }

void HandleNewSectorAmbience(uint8_t ubAmbientID) {
  // OK, we could have just loaded a sector, erase all ambient sounds from
  // queue, shutdown all ambient groupings
  SoundStopAllRandom();

  DeleteAllStrategicEventsOfType(EVENT_AMBIENT);

  if (!gfBasement && !gfCaves) {
    if (LoadAmbientControlFile(ubAmbientID)) {
      // OK, load them up!
      BuildDayAmbientSounds();
    } else {
      DebugMsg(TOPIC_JA2, DBG_LEVEL_0, "Cannot load Ambient data for tileset");
    }
  }
}

void DeleteAllAmbients() {
  // JA2Gold: it seems that ambient sounds don't get unloaded when we exit a
  // sector!?
  SoundStopAllRandom();
  DeleteAllStrategicEventsOfType(EVENT_AMBIENT);
}

uint32_t SetupNewAmbientSound(uint32_t uiAmbientID) {
  const AMBIENTDATA_STRUCT *const a = &gAmbData[uiAmbientID];
  const uint32_t vol = CalculateSoundEffectsVolume(a->uiVol);
  return SoundPlayRandom(a->zFilename, a->uiMinTime, a->uiMaxTime, vol, vol, MIDDLEPAN, MIDDLEPAN,
                         1);
}

#include "gtest/gtest.h"

TEST(AmbientControl, asserts) { EXPECT_EQ(sizeof(AMBIENTDATA_STRUCT), 116); }
