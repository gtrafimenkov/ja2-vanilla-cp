#include <exception>
#include <new>

#include "slog/slog.h"

#include "Button_System.h"
#include "Debug.h"
#include "FileMan.h"
#include "Font.h"
#include "GameLoop.h"
#include "Init.h" // XXX should not be used in SGP
#include "Input.h"
#include "Intro.h"
#include "JA2_Splash.h"
#include "MemMan.h"
#include "Random.h"
#include "SGP.h"
#include "SaveLoadGame.h" // XXX should not be used in SGP
#include "SoundMan.h"
#include "VObject.h"
#include "Video.h"
#include "VSurface.h"
#include <SDL.h>
#include "UILayout.h"
#include "GameRes.h"
#include "Logger.h"
#include "GameState.h"
#include "Exceptions.h"
#include "Timer.h"

#ifdef WITH_UNITTESTS
#include "gtest/gtest.h"
#endif

#if defined _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>

#	include "Local.h"
#endif

#include "Build/Utils/Multi_Language_Graphic_Utils.h"


extern BOOLEAN gfPauseDueToPlayerGamePause;


#define WITH_MODS (1)

////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////

// #include "src/JsonObject.h"
// #include "src/MagazineModel.h"
// #include "src/WeaponModels.h"
// #include "Build/Tactical/Weapons.h"
// #include "rapidjson/document.h"
// #include "rapidjson/filestream.h"
// #include "rapidjson/prettywriter.h"
// #include "stdio.h"

// bool writeWeaponsToJson(const char *name/*, const struct WEAPONTYPE *weapon*/, int weaponCount)
// {
//   FILE *f = fopen(name, "wt");
//   if(f)
//   {
//     rapidjson::FileStream os(f);
//     rapidjson::PrettyWriter<rapidjson::FileStream> writer(os);

//     rapidjson::Document document;
//     document.SetArray();
//     rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

//     for(int i = 0; i < weaponCount; i++)
//     {
//       // printf("%d\n", i);
//       const WeaponModel *w = GCM->getWeapon(i);
//       JsonObject obj(allocator);
//       w->serializeTo(obj);
//       document.PushBack(obj.getValue(), allocator);
//     }

//     document.Accept(writer);

//     fputs("\n", f);
//     return fclose(f) == 0;
//   }
//   return false;
// }

// bool writeMagazinesToJson(const char *name)
// {
//   FILE *f = fopen(name, "wt");
//   if(f)
//   {
//     rapidjson::FileStream os(f);
//     rapidjson::PrettyWriter<rapidjson::FileStream> writer(os);

//     rapidjson::Document document;
//     document.SetArray();
//     rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

//     const std::vector<const MagazineModel*>& magazines = GCM->getMagazines();
//     for(const MagazineModel* mag: magazines)
//     {
//       JsonObject obj(allocator);
//       mag->serializeTo(obj);
//       document.PushBack(obj.getValue(), allocator);
//     }

//     document.Accept(writer);

//     fputs("\n", f);
//     return fclose(f) == 0;
//   }
//   return false;
// }

////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////
/**
 * Number of milliseconds for one game cycle.
 * 25 ms gives approx. 40 cycles per second (and 40 frames per second, since the screen
 * is updated on every cycle). */
#define MS_PER_GAME_CYCLE               (25)


static BOOLEAN gfGameInitialized = FALSE;


static void InitializeStandardGamingPlatform(void)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_EnableUNICODE(SDL_ENABLE);

#ifdef SGP_DEBUG
	// Initialize the Debug Manager - success doesn't matter
	InitializeDebugManager();
#endif

  // this one needs to go ahead of all others (except Debug), for MemDebugCounter to work right...
	FastDebugMsg("Initializing Memory Manager");
	// Initialize the Memory Manager
	InitializeMemoryManager();

	FastDebugMsg("Initializing File Manager");
	InitializeFileManager();

	FastDebugMsg("Initializing Video Manager");
	InitializeVideoManager();

	FastDebugMsg("Initializing Video Object Manager");
	InitializeVideoObjectManager();

	FastDebugMsg("Initializing Video Surface Manager");
	InitializeVideoSurfaceManager();

  InitGameResources();

#ifdef JA2
	InitJA2SplashScreen();
#endif

	// Initialize Font Manager
	FastDebugMsg("Initializing the Font Manager");
	// Init the manager and copy the TransTable stuff into it.
	InitializeFontManager();

	FastDebugMsg("Initializing Sound Manager");
#ifndef UTIL
	InitializeSoundManager();
#endif

	FastDebugMsg("Initializing Random");
  // Initialize random number generator
  InitializeRandom(); // no Shutdown

	FastDebugMsg("Initializing Game Manager");
	// Initialize the Game
	InitializeGame();

	gfGameInitialized = TRUE;
}


/** Deinitialize the game an exit. */
static void deinitGameAndExit()
{
	FastDebugMsg("Exiting Game");

	SoundServiceStreams();

	if (gfGameInitialized)
	{
		ShutdownGame();
	}

	ShutdownButtonSystem();
	MSYS_Shutdown();

#ifndef UTIL
  ShutdownSoundManager();
#endif

	ShutdownVideoSurfaceManager();
  ShutdownVideoObjectManager();
  ShutdownVideoManager();

  ShutdownMemoryManager();  // must go last, for MemDebugCounter to work right...

  SDL_Quit();

  exit(0);
}


/** Request game exit.
 * Call this function if you want to exit the game. */
void requestGameExit()
{
  SDL_Event event;
  event.type = SDL_QUIT;
  SDL_PushEvent(&event);
}

static void MainLoop()
{
	BOOLEAN s_doGameCycles = TRUE;

  while (true)
  {
    // cycle until SDL_Quit is received

		SDL_Event event;
		if (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_APP_WILLENTERBACKGROUND:
					s_doGameCycles = false;
					break;

				case SDL_APP_WILLENTERFOREGROUND:
					s_doGameCycles = true;
					break;

				case SDL_KEYDOWN: KeyDown(&event.key.keysym); break;
				case SDL_KEYUP:   KeyUp(  &event.key.keysym); break;
				case SDL_TEXTINPUT: TextInput(&event.text); break;

				case SDL_MOUSEBUTTONDOWN: MouseButtonDown(&event.button); break;
				case SDL_MOUSEBUTTONUP:   MouseButtonUp(&event.button);   break;

				case SDL_MOUSEMOTION:
					SetSafeMousePosition(event.motion.x, event.motion.y);
					break;

				case SDL_MOUSEWHEEL: MouseWheelScroll(&event.wheel); break;

				case SDL_QUIT: deinitGameAndExit(); break;
			}
		}
		else
		{
			if (s_doGameCycles)
			{
        UINT32 gameCycleMS = GetClock();
#if DEBUG_PRINT_GAME_CYCLE_TIME
        UINT32 totalGameCycleMS = gameCycleMS;
#endif
				GameLoop();
        gameCycleMS = GetClock() - gameCycleMS;

        if(gameCycleMS < MS_PER_GAME_CYCLE)
        {
          SDL_Delay(MS_PER_GAME_CYCLE - gameCycleMS);
        }

#if DEBUG_PRINT_GAME_CYCLE_TIME
        totalGameCycleMS = GetClock() - totalGameCycleMS;
        printf("game cycle: %4d %4d\n", gameCycleMS, totalGameCycleMS);
#endif
			}
			else
			{
				SDL_WaitEvent(NULL);
			}
		}
  }
}


static int Failure(char const* const msg, bool showInfoIcon=false)
{
	fprintf(stderr, "%s\n", msg);
#if defined _WIN32
	MessageBox(0, msg, APPLICATION_NAME, MB_OK | (showInfoIcon ? MB_ICONINFORMATION : MB_ICONERROR) | MB_TASKMODAL);
#endif
	return EXIT_FAILURE;
}

////////////////////////////////////////////////////////////

struct CommandLineParams
{
  CommandLineParams()
  {
    doUnitTests = false;
    showDebugMessages = false;
    resourceVersionGiven = false;
    gameDirPathGiven = false;
  }

  bool resourceVersionGiven;
  std::string resourceVersion;

  bool doUnitTests;
  bool showDebugMessages;

  bool gameDirPathGiven;
  std::string gameDirPath;
};

static BOOLEAN ParseParameters(int argc, char* const argv[],
                               bool *doUnitTests,
                               bool *showDebugMessages);


int main(int argc, char* argv[])
try
{
  std::string exeFolder = FileMan::getParentPath(argv[0], true);
#if defined BROKEN_SWPRINTF
	if (setlocale(LC_CTYPE, "UTF-8") == NULL)
	{
		fprintf(stderr, "WARNING: Failed to set LC_CTYPE to UTF-8. Some strings might get garbled.\n");
	}
#endif

  // init logging
  SLOG_Init(SLOG_STDERR, NULL);
  SLOG_SetLevel(SLOG_WARNING, SLOG_WARNING);

  setGameVersion(GV_ENGLISH);

  bool doUnitTests = false;
  bool showDebugMessages = false;
	if (!ParseParameters(argc, argv, &doUnitTests, &showDebugMessages)) return EXIT_FAILURE;

  if(showDebugMessages)
  {
    SLOG_SetLevel(SLOG_DEBUG, SLOG_DEBUG);
  }

#ifdef WITH_UNITTESTS
  if(doUnitTests)
  {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  }
#endif

  GameVersion version = GV_ENGLISH;
  if(params.resourceVersionGiven)
  {
    if(!getResourceVersion(params.resourceVersion.c_str(), &version))
    {
      SLOGE(TAG, "Unknown version of the game: %s\n", params.resourceVersion.c_str());
      return EXIT_FAILURE;
    }
  }
  setGameVersion(version);

  ////////////////////////////////////////////////////////////

	SDL_Init(SDL_INIT_VIDEO);

  // restore output to the console (on windows when built with MINGW)
#ifdef __MINGW32__
  freopen("CON", "w", stdout);
  freopen("CON", "w", stderr);
#endif

#ifdef SGP_DEBUG
	// Initialize the Debug Manager - success doesn't matter
	InitializeDebugManager();
#endif

  // this one needs to go ahead of all others (except Debug), for MemDebugCounter to work right...
	FastDebugMsg("Initializing Memory Manager");
	InitializeMemoryManager();

  FastDebugMsg("Initializing Game Resources");
  if(!params.gameDirPathGiven) {
    params.gameDirPath = exeFolder;
  }

  std::string extraDataDir = exeFolder;

  std::string externalizedDataPath = FileMan::joinPaths(extraDataDir, "externalized");

  DefaultContentManager *cm;

  {
    cm = new DefaultContentManager(version,
                                   configFolderPath, configPath,
                                   gameResRootPath, externalizedDataPath);
    LOG_INFO("------------------------------------------------------------------------------\n");
    LOG_INFO("Root game resources directory: '%s'\n", params.gameDirPath.c_str());
    LOG_INFO("Extra data directory:          '%s'\n", extraDataDir.c_str());
    LOG_INFO("Data directory:                '%s'\n", cm->getDataDir().c_str());
    LOG_INFO("Tilecache directory:           '%s'\n", cm->getTileDir().c_str());
    LOG_INFO("Saved games directory:         '%s'\n", cm->getSavedGamesFolder().c_str());
    LOG_INFO("------------------------------------------------------------------------------\n");
  }

	// InitializeStandardGamingPlatform();

  if(!cm->loadGameData())

#if defined JA2
  if(isEnglishVersion())
  {
    SetIntroType(INTRO_SPLASH);
  }
  else
  {

    GCM = cm;

    FastDebugMsg("Initializing Video Manager");
    InitializeVideoManager();

    FastDebugMsg("Initializing Video Object Manager");
    InitializeVideoObjectManager();

    FastDebugMsg("Initializing Video Surface Manager");
    InitializeVideoSurfaceManager();

    InitJA2SplashScreen();

	FastDebugMsg("Running Game");

    FastDebugMsg("Initializing Sound Manager");
    InitializeSoundManager();

    FastDebugMsg("Initializing Random");
    // Initialize random number generator
    InitializeRandom(); // no Shutdown

    FastDebugMsg("Initializing Game Manager");
    // Initialize the Game
    InitializeGame();

    gfGameInitialized = TRUE;

    ////////////////////////////////////////////////////////////

    // some data convertion
    // convertDialogQuotesToJson(cm, SE_RUSSIAN, "mercedt/051.edt", FileMan::joinPaths(exeFolder, "051.edt.json").c_str());
    // convertDialogQuotesToJson(cm, SE_RUSSIAN, "mercedt/052.edt", FileMan::joinPaths(exeFolder, "052.edt.json").c_str());
    // convertDialogQuotesToJson(cm, SE_RUSSIAN, "mercedt/055.edt", FileMan::joinPaths(exeFolder, "055.edt.json").c_str());

    // writeWeaponsToJson(FileMan::joinPaths(exeFolder, "externalized/weapons.json").c_str(), MAX_WEAPONS+1);
    // writeMagazinesToJson(FileMan::joinPaths(exeFolder, "externalized/magazines.json").c_str());

    // readWeaponsFromJson(FileMan::joinPaths(exeFolder, "weapon.json").c_str());
    // readWeaponsFromJson(FileMan::joinPaths(exeFolder, "weapon2.json").c_str());

    ////////////////////////////////////////////////////////////

    if(isEnglishVersion())
    {
      SetIntroType(INTRO_SPLASH);
    }

    FastDebugMsg("Running Game");

    /* At this point the SGP is set up, which means all I/O, Memory, tools, etc.
     * are available. All we need to do is attend to the gaming mechanics
     * themselves */
    MainLoop(GCM->getGamePolicy()->ms_per_game_cycle);
  }

	/* At this point the SGP is set up, which means all I/O, Memory, tools, etc.
	 * are available. All we need to do is attend to the gaming mechanics
	 * themselves */
	MainLoop();

  SLOG_Deinit();

	return EXIT_SUCCESS;
}
catch (const std::bad_alloc&)
{
	return Failure("ERROR: out of memory");
}
catch (const LibraryFileNotFoundException& e)
{
	return Failure(e.what(), true);
}
catch (const std::exception& e)
{
	char msg[2048];
	snprintf(msg, lengthof(msg), "ERROR: caught unhandled exception:\n%s", e.what());
	return Failure(msg);
}
catch (...)
{
	return Failure("ERROR: caught unhandled unknown exception");
}


/** Set game resources version. */
static BOOLEAN setResourceVersion(const char *version)
{
  if(strcasecmp(version, "ENGLISH") == 0)
  {
    setGameVersion(GV_ENGLISH);
  }
  else if(strcasecmp(version, "DUTCH") == 0)
  {
    setGameVersion(GV_DUTCH);
  }
  else if(strcasecmp(version, "FRENCH") == 0)
  {
    setGameVersion(GV_FRENCH);
  }
  else if(strcasecmp(version, "GERMAN") == 0)
  {
    setGameVersion(GV_GERMAN);
  }
  else if(strcasecmp(version, "ITALIAN") == 0)
  {
    setGameVersion(GV_ITALIAN);
  }
  else if(strcasecmp(version, "POLISH") == 0)
  {
    setGameVersion(GV_POLISH);
  }
  else if(strcasecmp(version, "RUSSIAN") == 0)
  {
    setGameVersion(GV_RUSSIAN);
  }
  else if(strcasecmp(version, "RUSSIAN_GOLD") == 0)
  {
    setGameVersion(GV_RUSSIAN_GOLD);
  }
  else
  {
    LOG_ERROR("Unknown version of the game: %s\n", version);
    return false;
  }
  LOG_INFO("Game version: %s\n", version);
  return true;
}

static BOOLEAN ParseParameters(int argc, char* const argv[],
                               bool *doUnitTests,
                               bool *showDebugMessages)
{
	const char* const name = *argv;
	if (name == NULL) return TRUE; // argv does not even contain the program name

	BOOLEAN success = TRUE;
  for(int i = 1; i < argc; i++)
  {
    bool haveNextParameter = (i + 1) < argc;

		if (strcmp(argv[i], "--nosound") == 0)
		{
			SoundEnableSound(FALSE);
		}
#ifdef WITH_UNITTESTS
    else if (strcmp(argv[i], "--unittests") == 0)
    {
      *doUnitTests = true;
      return true;
    }
#endif
    else if (strcmp(argv[i], "--debug") == 0)
    {
      *showDebugMessages = true;
      return true;
    }
#ifdef WITH_MODS
    else if (strcmp(argv[i], "--mod") == 0)
    {
      if(haveNextParameter)
      {
        params->useMod = true;
        params->modName = argv[++i];
      }
      else
      {
        LOG_ERROR("Missing value for command-line key '--mod'\n");
        success = FALSE;
      }
    }
#endif
    else if (strcmp(argv[i], "--gamedir") == 0)
    {
      if(haveNextParameter)
      {
        params->gameDirPathGiven = true;
        params->gameDirPath = argv[++i];
      }
      else
      {
        LOG_ERROR("Missing value for command-line key '-gamedir'\n");
        success = FALSE;
      }
    }
		else if (strcmp(argv[i], "--editor") == 0)
		}
#if defined JA2BETAVERSION
		else if (strcmp(argv[i], "-quicksave") == 0)
		{
			/* This allows the QuickSave Slots to be autoincremented, i.e. everytime
			 * the user saves, there will be a new quick save file */
			gfUseConsecutiveQuickSaveSlots = TRUE;
		}
		else if (strcmp(argv[i], "-domaps") == 0)
		{
      GameState::setMode(GAME_MODE_MAP_UTILITY);
		}
#endif
		else if (strcmp(argv[i], "-editor") == 0)
		{
      GameState::getInstance()->setEditorMode(false);
		}
		else if (strcmp(argv[i], "--editorauto") == 0)
		{
      GameState::getInstance()->setEditorMode(true);
		}
    else if (strcmp(argv[i], "--resversion") == 0)
    {
      if(haveNextParameter)
      {
        success = setResourceVersion(argv[++i]);
      }
      else
      {
        LOG_ERROR("Missing value for command-line key '-resversion'\n");
        success = FALSE;
      }
    }
		else
		{
			if (strcmp(argv[i], "--help") != 0)
			{
				fprintf(stderr, "Unknown switch \"%s\"\n", argv[i]);
			}
			success = FALSE;
		}
	}

	if (!success)
	{
		fprintf(stderr,
			"Usage: %s [options]\n"
			"\n"
			"  --resversion  Version of the game resources.\n"
			"                Possible values: DUTCH, ENGLISH, FRENCH, GERMAN, ITALIAN, POLISH, RUSSIAN, RUSSIAN_GOLD\n"
			"                Default value is ENGLISH\n"
			"                RUSSIAN is for BUKA Agonia Vlasty release\n"
			"                RUSSIAN_GOLD is for Gold release\n"
			"\n"
      "  --gamedir     Directory where the original game resources can be found.\n"
      "                By default the directory where the executable file is located.\n"
			"\n"
			"  --debug       Show debug messages\n"
#ifdef WITH_UNITTESTS
			"\n"
      "  --unittests   Perform unit tests\n"
      "                  ja2-ve --unittests [gtest options]\n"
      "                  E.g. ja2-ve --unittests --gtest_output=\"xml:report.xml\" --gtest_repeat=2\n"
			"\n"
#endif
			"  --editor      Start the map editor (Editor.slf is required)\n"
			"  --editorauto  Start the map editor and load sector A9 (Editor.slf is required)\n"
			"  --help        Display this information\n"
			"  --nosound     Turn the sound and music off\n"
            ,
			name
		);
	}
	return success;
}
