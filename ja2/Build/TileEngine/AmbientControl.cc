#include "TileEngine/AmbientTypes.h"
#include "Directories.h"
#include "SGP/FileMan.h"
#include "TileEngine/Environment.h"
#include "Utils/SoundControl.h"
#include "Strategic/GameEvents.h"
#include "TileEngine/AmbientControl.h"
#include "SGP/SoundMan.h"
#include "SGP/Debug.h"
#include "SGP/StrUtils.h"

AMBIENTDATA_STRUCT		gAmbData[ MAX_AMBIENT_SOUNDS ];
INT16									gsNumAmbData = 0;

static BOOLEAN LoadAmbientControlFile(UINT8 ubAmbientID)
try
{
	std::string zFilename = FormattedString(AMBIENTDIR "/%d.bad", ubAmbientID);

	AutoSGPFile hFile(FileMan::openForReadingSmart(zFilename.c_str(), true));

	// READ #
	FileRead(hFile, &gsNumAmbData, sizeof(INT16));

	// LOOP FOR OTHERS
	for (INT32 cnt = 0; cnt < gsNumAmbData; cnt++)
	{
		FileRead(hFile, &gAmbData[cnt], sizeof(AMBIENTDATA_STRUCT));
    std::string filename = FormattedString(AMBIENTDIR "/%s", gAmbData[cnt].zFilename);
    strcpy(gAmbData[cnt].zFilename, filename.c_str());
	}

	return TRUE;
}
catch (...) { return FALSE; }


void StopAmbients( )
{
	SoundStopAllRandom( );
}

void HandleNewSectorAmbience( UINT8 ubAmbientID )
{
	// OK, we could have just loaded a sector, erase all ambient sounds from queue, shutdown all ambient groupings
	SoundStopAllRandom( );

	DeleteAllStrategicEventsOfType( EVENT_AMBIENT );

	if( !gfBasement && !gfCaves )
	{
		if(	LoadAmbientControlFile( ubAmbientID ) )
		{
			// OK, load them up!
			BuildDayAmbientSounds( );
		}
		else
		{
			DebugMsg(TOPIC_JA2, DBG_LEVEL_0, "Cannot load Ambient data for tileset");
		}
	}
}

void DeleteAllAmbients()
{
	// JA2Gold: it seems that ambient sounds don't get unloaded when we exit a sector!?
	SoundStopAllRandom();
	DeleteAllStrategicEventsOfType( EVENT_AMBIENT );
}


UINT32 SetupNewAmbientSound( UINT32 uiAmbientID )
{
	const AMBIENTDATA_STRUCT* const a   = &gAmbData[uiAmbientID];
	const UINT32                    vol = CalculateSoundEffectsVolume(a->uiVol);
	return SoundPlayRandom(a->zFilename, a->uiMinTime, a->uiMaxTime, vol, vol, MIDDLEPAN, MIDDLEPAN, 1);
}


#include "gtest/gtest.h"

TEST(AmbientControl, asserts)
{
  EXPECT_EQ(sizeof(AMBIENTDATA_STRUCT), 116);
}
