#include "Editor/EditScreen.h"
#include "Editor/SummaryInfo.h"
#include "GameSettings.h"
#include "GameState.h"
#include "Init.h"
#include "JAScreens.h"
#include "Laptop/Laptop.h"
#include "Local.h"
#include "Screens.h"
#include "Strategic/GameInit.h"
#include "Strategic/StrategicMovementCosts.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicMap.h"
#include "SysGlobals.h"
#include "Tactical/AnimationData.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/MapInformation.h"
#include "Tactical/Overhead.h"
#include "Tactical/Vehicles.h"
#include "TacticalAI/NPC.h"
#include "TileEngine/ExitGrids.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/TileCache.h"
#include "TileEngine/WorldDef.h"
#include "Utils/EventManager.h"
#include "Utils/FontControl.h"
#include "Utils/MercTextBox.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"
#include "SGP/CursorControl.h"
#include "SGP/HImage.h"
#include "SGP/MouseSystem.h"
#include "SGP/Shading.h"
#include "SGP/Video.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"


// The InitializeGame function is responsible for setting up all data and Gaming Engine
// tasks which will run the game


ScreenID InitializeJA2(void)
try
{
	gfWorldLoaded = FALSE;

	// Load external text
	LoadAllExternalText();

	gsRenderCenterX = 805;
	gsRenderCenterY = 805;

	// Init animation system
	InitAnimationSystem();

	// Init lighting system
	InitLightingSystem();

	// Init dialog queue system
	InitalizeDialogueControl();

	InitStrategicEngine();

	//needs to be called here to init the SectorInfo struct
	InitStrategicMovementCosts( );

	// Init tactical engine
	InitTacticalEngine();

	// Init timer system
	//Moved to the splash screen code.
	//InitializeJA2Clock( );

	// INit shade tables
	BuildShadeTable( );

	// INit intensity tables
	BuildIntensityTable( );

	// Init Event Manager
	InitializeEventManager();

	// Initailize World
	InitializeWorld();

	InitTileCache( );

	InitMercPopupBox( );

  if(GameState::getInstance()->isEditorMode())
  {
    //UNCOMMENT NEXT LINE TO ALLOW FORCE UPDATES...
    //LoadGlobalSummary();
    if( gfMustForceUpdateAllMaps )
    {
      ApologizeOverrideAndForceUpdateEverything();
    }
  }

	switch (GameState::getInstance()->getMode())
	{
		case GAME_MODE_EDITOR:
			DebugMsg(TOPIC_JA2EDITOR, DBG_LEVEL_1, "Beginning JA2 using -editor commandline argument...");
			gfAutoLoadA9 = FALSE;
			goto editor;

		case GAME_MODE_EDITOR_AUTO:
			DebugMsg(TOPIC_JA2EDITOR, DBG_LEVEL_1, "Beginning JA2 using -editorauto commandline argument...");
			gfAutoLoadA9 = TRUE;
editor:
			//For editor purposes, need to know the default map file.
			strcpy(g_filename, "none");
			//also set the sector
			SetWorldSectorInvalid();
			gfIntendOnEnteringEditor = TRUE;
			gGameOptions.fGunNut     = TRUE;
			return GAME_SCREEN;

		default: return INIT_SCREEN;
	}
}
catch (...) { return ERROR_SCREEN; }


void ShutdownJA2(void)
{
  UINT32 uiIndex;

	FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));
	InvalidateScreen( );
	// Remove cursor....
	SetCurrentCursorFromDatabase( VIDEO_NO_CURSOR );

	RefreshScreen();

	ShutdownStrategicLayer();

	// remove temp files built by laptop
	ClearOutTempLaptopFiles( );

	// Shutdown queue system
	ShutdownDialogueControl();

  // Shutdown Screens
  for (uiIndex = 0; uiIndex < MAX_SCREENS; uiIndex++)
  {
		void (*const shutdown)(void) = GameScreens[uiIndex].ShutdownScreen;
		if (shutdown != NULL) shutdown();
  }


	ShutdownLightingSystem();

	CursorDatabaseClear();

	ShutdownTacticalEngine( );

	// Shutdown Overhead
	ShutdownOverhead( );

	DeInitAnimationSystem();

	DeinitializeWorld( );

	DeleteTileCache( );

	ShutdownJA2Clock( );

	ShutdownFonts();

	ShutdownJA2Sound( );

	ShutdownEventManager( );

	ClearOutVehicleList();
}
