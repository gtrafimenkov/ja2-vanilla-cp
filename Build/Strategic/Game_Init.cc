#include <stdexcept>

#include "Build/Utils/Font_Control.h"
#include "Build/JAScreens.h"
#include "Build/Laptop/Laptop.h"
#include "Build/Strategic/MapScreen.h"
#include "Build/Strategic/Meanwhile.h"
#include "Build/Tactical/Merc_Hiring.h"
#include "Build/Strategic/Queen_Command.h"
#include "Build/Tactical/ShopKeeper_Interface.h"
#include "Build/Utils/Timer_Control.h"
#include "Build/TileEngine/WorldDef.h"
#include "Build/Tactical/Soldier_Control.h"
#include "Build/Tactical/Overhead.h"
#include "Build/Laptop/EMail.h"
#include "Build/Strategic/Game_Clock.h"
#include "Build/Tactical/Soldier_Profile.h"
#include "Build/Strategic/StrategicMap.h"
#include "Build/Strategic/Game_Init.h"
#include "Build/Tactical/Animation_Data.h"
#include "Build/Laptop/Finances.h"
#include "Build/Tactical/Soldier_Create.h"
#include "Build/Tactical/Soldier_Init_List.h"
#include "Build/Strategic/Strategic.h"
#include "Build/Laptop/History.h"
#include "Build/Tactical/Merc_Entering.h"
#include "Build/Tactical/Squads.h"
#include "Build/Strategic/Campaign_Init.h"
#include "Build/Strategic/Strategic_Town_Loyalty.h"
#include "Build/Strategic/Strategic_Mines.h"
#include "Build/GameLoop.h"
#include "sgp/Random.h"
#include "Build/Strategic/Map_Screen_Interface.h"
#include "Build/Tactical/Tactical_Save.h"
#include "Build/Strategic/Campaign_Types.h"
#include "Build/Utils/Message.h"
#include "Build/Strategic/Game_Event_Hook.h"
#include "Build/Strategic/Strategic_Movement.h"
#include "Build/Strategic/Creature_Spreading.h"
#include "Build/Strategic/Quests.h"
#include "Build/Strategic/Strategic_AI.h"
#include "Build/Laptop/LaptopSave.h"
#include "Build/Laptop/AIMMembers.h"
#include "Build/Tactical/Dialogue_Control.h"
#include "Build/TacticalAI/NPC.h"
#include "Build/Tactical/OppList.h"
#include "Build/GameSettings.h"
#include "Build/Tactical/Interface_Dialogue.h"
#include "Build/Strategic/Map_Screen_Interface_Border.h"
#include "Build/Strategic/Map_Screen_Helicopter.h"
#include "Build/Tactical/Vehicles.h"
#include "Build/Strategic/Map_Screen_Interface_Map.h"
#include "Build/Strategic/PreBattle_Interface.h"
#include "Build/Tactical/Arms_Dealer_Init.h"
#include "Build/Laptop/BobbyR.h"
#include "Build/HelpScreen.h"
#include "Build/Tactical/Air_Raid.h"
#include "Build/Tactical/Interface.h"
#include "Build/Cheats.h"
#include "sgp/SoundMan.h"
#include "sgp/Debug.h"
#include "Build/ScreenIDs.h"

#include "src/ContentManager.h"
#include "src/GameInstance.h"
#include "src/policy/GamePolicy.h"

UINT8			gubScreenCount=0;


static void InitNPCs(void)
{
	{ // add the pilot at a random location!
		MERCPROFILESTRUCT& p = GetProfile(SKYRIDER);
		switch (Random(4))
		{
			case 0:
				p.sSectorX = 15;
				p.sSectorY = MAP_ROW_B;
				p.bSectorZ = 0;
				break;
			case 1:
				p.sSectorX = 14;
				p.sSectorY = MAP_ROW_E;
				p.bSectorZ = 0;
				break;
			case 2:
				p.sSectorX = 12;
				p.sSectorY = MAP_ROW_D;
				p.bSectorZ = 0;
				break;
			case 3:
				p.sSectorX = 16;
				p.sSectorY = MAP_ROW_C;
				p.bSectorZ = 0;
				break;
		}

		// use alternate map, with Skyrider's shack, in this sector
		SectorInfo[SECTOR(p.sSectorX, p.sSectorY)].uiFlags |= SF_USE_ALTERNATE_MAP;
	}


	// set up Madlab's secret lab (he'll be added when the meanwhile scene occurs)

	switch( Random( 4 ) )
	{
		case 0:
			// use alternate map in this sector
			SectorInfo[ SECTOR( 7, MAP_ROW_H ) ].uiFlags |= SF_USE_ALTERNATE_MAP;
			break;
		case 1:
			SectorInfo[ SECTOR( 16, MAP_ROW_H ) ].uiFlags |= SF_USE_ALTERNATE_MAP;
			break;
		case 2:
			SectorInfo[ SECTOR( 11, MAP_ROW_I ) ].uiFlags |= SF_USE_ALTERNATE_MAP;
			break;
		case 3:
			SectorInfo[ SECTOR( 4, MAP_ROW_E ) ].uiFlags |= SF_USE_ALTERNATE_MAP;
			break;
	}

	{ // add Micky in random location
		MERCPROFILESTRUCT& p = GetProfile(MICKY);
		switch (Random(5))
		{
			case 0:
				p.sSectorX = 9;
				p.sSectorY = MAP_ROW_G;
				p.bSectorZ = 0;
				break;
			case 1:
				p.sSectorX = 13;
				p.sSectorY = MAP_ROW_D;
				p.bSectorZ = 0;
				break;
			case 2:
				p.sSectorX = 5;
				p.sSectorY = MAP_ROW_C;
				p.bSectorZ = 0;
				break;
			case 3:
				p.sSectorX = 2;
				p.sSectorY = MAP_ROW_H;
				p.bSectorZ = 0;
				break;
			case 4:
				p.sSectorX = 6;
				p.sSectorY = MAP_ROW_C;
				p.bSectorZ = 0;
				break;
		}

		// use alternate map in this sector
		//SectorInfo[SECTOR(p.sSectorX, p.sSectorY)].uiFlags |= SF_USE_ALTERNATE_MAP;
	}

  gfPlayerTeamSawJoey = FALSE;


	if ( gGameOptions.fSciFi )
	{
		{ // add Bob
			MERCPROFILESTRUCT& p = GetProfile(BOB);
			p.sSectorX = 8;
			p.sSectorY = MAP_ROW_F;
			p.bSectorZ = 0;
		}

		{ // add Gabby in random location
			MERCPROFILESTRUCT& p = gMercProfiles[GABBY];
			switch( Random( 2 ) )
			{
				case 0:
					p.sSectorX = 11;
					p.sSectorY = MAP_ROW_H;
					p.bSectorZ = 0;
					break;
				case 1:
					p.sSectorX = 4;
					p.sSectorY = MAP_ROW_I;
					p.bSectorZ = 0;
					break;
			}

			// use alternate map in this sector
			SectorInfo[SECTOR(p.sSectorX, p.sSectorY)].uiFlags |= SF_USE_ALTERNATE_MAP;
		}
	}
	else
	{ //not scifi, so use alternate map in Tixa's b1 level that doesn't have the stairs going down to the caves.
		UNDERGROUND_SECTORINFO *pSector;
		pSector = FindUnderGroundSector( 9, 10, 1 ); //j9_b1
		if( pSector )
		{
			pSector->uiFlags |= SF_USE_ALTERNATE_MAP;
		}
	}

	// init hospital variables
	giHospitalTempBalance = 0;
	giHospitalRefund = 0;
	gbHospitalPriceModifier = 0;

	// set up Devin so he will be placed ASAP
	gMercProfiles[ DEVIN ].bNPCData = 3;
}

void InitBloodCatSectors()
{
	INT32 i;
	//Hard coded table of bloodcat populations.  We don't have
	//access to the real population (if different) until we physically
	//load the map.  If the real population is different, then an error
	//will be reported.
	for( i = 0; i < 255; i++ )
	{
		SectorInfo[ i ].bBloodCats = -1;
	}
	SectorInfo[ SEC_A15	].bBloodCatPlacements = 9;
	SectorInfo[ SEC_B4	].bBloodCatPlacements = 9;
	SectorInfo[ SEC_B16	].bBloodCatPlacements = 8;
	SectorInfo[ SEC_C3	].bBloodCatPlacements = 12;
	SectorInfo[ SEC_C8	].bBloodCatPlacements = 13;
	SectorInfo[ SEC_C11	].bBloodCatPlacements = 7;
	SectorInfo[ SEC_D4	].bBloodCatPlacements = 8;
	SectorInfo[ SEC_D9	].bBloodCatPlacements = 12;
	SectorInfo[ SEC_E11	].bBloodCatPlacements = 10;
	SectorInfo[ SEC_E13	].bBloodCatPlacements = 14;
	SectorInfo[ SEC_F3	].bBloodCatPlacements = 13;
	SectorInfo[ SEC_F5	].bBloodCatPlacements = 7;
	SectorInfo[ SEC_F7	].bBloodCatPlacements = 12;
	SectorInfo[ SEC_F12	].bBloodCatPlacements = 9;
	SectorInfo[ SEC_F14	].bBloodCatPlacements = 14;
	SectorInfo[ SEC_F15	].bBloodCatPlacements = 8;
	SectorInfo[ SEC_G6	].bBloodCatPlacements = 7;
	SectorInfo[ SEC_G10	].bBloodCatPlacements = 12;
	SectorInfo[ SEC_G12	].bBloodCatPlacements = 11;
	SectorInfo[ SEC_H5	].bBloodCatPlacements = 9;
	SectorInfo[ SEC_I4	].bBloodCatPlacements = 8;
	SectorInfo[ SEC_I15	].bBloodCatPlacements = 8;
	SectorInfo[ SEC_J6	].bBloodCatPlacements = 11;
	SectorInfo[ SEC_K3	].bBloodCatPlacements = 12;
	SectorInfo[ SEC_K6	].bBloodCatPlacements = 14;
	SectorInfo[ SEC_K10	].bBloodCatPlacements = 12;
	SectorInfo[ SEC_K14	].bBloodCatPlacements = 14;

	switch( gGameOptions.ubDifficultyLevel )
	{
		case DIF_LEVEL_EASY: //50%
			SectorInfo[ SEC_I16	].bBloodCatPlacements = 14;
			SectorInfo[ SEC_I16	].bBloodCats = 14;
			SectorInfo[ SEC_N5	].bBloodCatPlacements = 8;
			SectorInfo[ SEC_N5	].bBloodCats = 8;
			break;
		case DIF_LEVEL_MEDIUM: //75%
			SectorInfo[ SEC_I16	].bBloodCatPlacements = 19;
			SectorInfo[ SEC_I16	].bBloodCats = 19;
			SectorInfo[ SEC_N5	].bBloodCatPlacements = 10;
			SectorInfo[ SEC_N5	].bBloodCats = 10;
			break;
		case DIF_LEVEL_HARD: //100%
			SectorInfo[ SEC_I16	].bBloodCatPlacements = 26;
			SectorInfo[ SEC_I16	].bBloodCats = 26;
			SectorInfo[ SEC_N5	].bBloodCatPlacements = 12;
			SectorInfo[ SEC_N5	].bBloodCats = 12;
			break;
	}
}


void InitStrategicLayer( void )
{
	// Clear starategic layer!
	SetupNewStrategicGame();
	InitQuestEngine();

	//Setup a new campaign via the enemy perspective.
	InitNewCampaign();
	// Init Squad Lists
	InitSquads();
	// Init vehicles
	InitVehicles( );
	// init town loyalty
	InitTownLoyalty();
	// init the mine management system
	InitializeMines();
	// initialize map screen flags
	InitMapScreenFlags();
	// initialize NPCs, select alternate maps, etc
	InitNPCs();
	// init Skyrider and his helicopter
	InitializeHelicopter();
	//Clear out the vehicle list
	ClearOutVehicleList();

	InitBloodCatSectors();

	InitializeSAMSites();

	// make Orta, Tixa, SAM sites not found
	InitMapSecrets();


	// free up any leave list arrays that were left allocated
	ShutDownLeaveList( );
	// re-set up leave list arrays for dismissed mercs
	InitLeaveList( );

	// reset time compression mode to X0 (this will also pause it)
	SetGameTimeCompressionLevel( TIME_COMPRESS_X0 );

	// Select the start sector as the initial selected sector
	ChangeSelectedMapSector(SECTORX(START_SECTOR), SECTORY(START_SECTOR), 0);

	// Reset these flags or mapscreen could be disabled and cause major headache.
	fDisableDueToBattleRoster = FALSE;
	fDisableMapInterfaceDueToBattle = FALSE;
}

void ShutdownStrategicLayer()
{
	DeleteAllStrategicEvents();
	RemoveAllGroups();
	TrashUndergroundSectorInfo();
	DeleteCreatureDirectives();
	KillStrategicAI();
}


void InitNewGame()
{
	uiMeanWhileFlags = 0;
	SetSelectedMan(0);

  resetCheatLevelToInitialValue();

	if (gubScreenCount == 0)
	{
		LoadMercProfiles();
		InitAllArmsDealers();
		InitBobbyRayInventory();
	}

	ClearTacticalMessageQueue();
	FreeGlobalMessageList(); // Clear mapscreen messages

	if (gubScreenCount == 0)
	{ // Our first time, go into laptop
		InitLaptopAndLaptopScreens();
		InitStrategicLayer();
		SetLaptopNewGameFlag();

		// This is for the "mercs climbing down from a rope" animation, NOT Skyrider
		ResetHeliSeats();

		UINT32 const now = GetWorldTotalMin();
		AddPreReadEmail(OLD_ENRICO_1, OLD_ENRICO_1_LENGTH, MAIL_ENRICO, now);
		AddPreReadEmail(OLD_ENRICO_2, OLD_ENRICO_2_LENGTH, MAIL_ENRICO, now);
		AddPreReadEmail(RIS_REPORT,   RIS_REPORT_LENGTH,   RIS_EMAIL,   now);
		AddPreReadEmail(OLD_ENRICO_3, OLD_ENRICO_3_LENGTH, MAIL_ENRICO, now);
		AddEmail(IMP_EMAIL_INTRO, IMP_EMAIL_INTRO_LENGTH, CHAR_PROFILE_SITE, now);

		// ATE: Set starting cash
		INT32 starting_cash;
		switch (gGameOptions.ubDifficultyLevel)
		{
			case DIF_LEVEL_EASY:   starting_cash = GCM->getGamePolicy()->starting_cash_easy; break;
			case DIF_LEVEL_MEDIUM: starting_cash = GCM->getGamePolicy()->starting_cash_medium; break;
			case DIF_LEVEL_HARD:   starting_cash = GCM->getGamePolicy()->starting_cash_hard; break;
			default: throw std::logic_error("invalid difficulty level");
		}
		AddTransactionToPlayersBook(ANONYMOUS_DEPOSIT, 0, now, starting_cash);

		// Schedule email for message from Speck at 7am 1 to 2 days in the future
		UINT32 const days_time_merc_site_available = Random(2) + 1;
		AddFutureDayStrategicEvent(EVENT_DAY3_ADD_EMAIL_FROM_SPECK, 60 * 7, 0, days_time_merc_site_available);

		SetLaptopExitScreen(INIT_SCREEN);
		SetPendingNewScreen(LAPTOP_SCREEN);
		gubScreenCount = 1;

		// Set the fact the game is in progress
		gTacticalStatus.fHasAGameBeenStarted = TRUE;
	}
	else if (gubScreenCount == 1)
	{
		gubScreenCount = 2;
	}
}


BOOLEAN AnyMercsHired( )
{
	CFOR_EACH_IN_TEAM(s, OUR_TEAM)
	{
		return TRUE;
	}
	return FALSE;
}


//This function is called when the game is REstarted.  Things that need to be reinited are placed in here
void ReStartingGame()
{
	//Pause the game
	gfGamePaused = TRUE;

	//Reset the sectors
	SetWorldSectorInvalid();

	SoundStopAll( );

	//we are going to restart a game so initialize the variable so we can initialize a new game
	gubScreenCount = 0;

	InitTacticalSave();

	//Loop through all the soldier and delete them all
	FOR_EACH_SOLDIER(i) TacticalRemoveSoldier(*i);

	// Re-init overhead...
	InitOverhead( );

	//Reset the email list
	ShutDownEmailList();

	//Reinit the laptopn screen variables
	InitLaptopAndLaptopScreens();
	LaptopScreenInit();

	//Reload the Merc profiles
	LoadMercProfiles( );

	// Reload quote files
	ReloadAllQuoteFiles();

	//Initialize the ShopKeeper Interface ( arms dealer inventory, etc. )
	ShopKeeperScreenInit();

	//Delete the world info
	TrashWorld();

	//Init the help screen system
	InitHelpScreenSystem();

	EmptyDialogueQueue( );

	if ( InAirRaid( ) )
	{
		EndAirRaid( );
	}

	//Make sure the game starts in the TEAM panel ( it wasnt being reset )
	gsCurInterfacePanel = TEAM_PANEL;

	//Delete all the strategic events
	DeleteAllStrategicEvents();

	//This function gets called when ur in a game a click the quit to main menu button, therefore no game is in progress
	gTacticalStatus.fHasAGameBeenStarted = FALSE;

	// Reset timer callbacks
	gpCustomizableTimerCallback = NULL;

  resetCheatLevelToInitialValue();
}
